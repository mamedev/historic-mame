/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

  Changes 2/27/99:
  	- added some rounding to the sorting of timers so that two timers
  		allocated to go off at the same time will go off in the order
  		they were allocated, without concern for floating point rounding
  		errors (thanks Juergen!)
  	- fixed a bug where the base_time was not updated when a CPU was
  		suspended, making subsequent calls to get_relative_time() return an
  		incorrect time (thanks Nicola!)
  	- changed suspended CPUs so that they don't eat their timeslice until
  		all other CPUs have used up theirs; this allows a slave CPU to
  		trigger a higher priority CPU in the middle of the timeslice
  	- added the ability to call timer_reset() on a oneshot or pulse timer
  		from within that timer's callback; in this case, the timer won't
  		get removed (oneshot) or won't get reprimed (pulse)

  Changes 12/17/99 (HJB):
	- added overclocking factor and functions to set/get it at runtime.

  Changes 12/23/99 (HJB):
	- added burn() function pointer to tell CPU cores when we want to
	  burn cycles, because the cores might need to adjust internal
	  counters or timers.

***************************************************************************/

#include "cpuintrf.h"
#include "driver.h"
#include "timer.h"
#include <math.h>


#define MAX_TIMERS 256


#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif





/*-------------------------------------------------
	internal timer structure
-------------------------------------------------*/

struct _mame_timer
{
	struct _mame_timer *next;
	struct _mame_timer *prev;
	void (*callback)(int);
	int callback_param;
	int tag;
	const char *file;
	int line;
	UINT8 enabled;
	UINT8 temporary;
	mame_time period;
	mame_time start;
	mame_time expire;
};



/*-------------------------------------------------
	global variables
-------------------------------------------------*/

/* conversion constants */
subseconds_t subseconds_per_cycle[MAX_CPU];
UINT32 cycles_per_second[MAX_CPU];
double cycles_to_sec[MAX_CPU];
double sec_to_cycles[MAX_CPU];

/* list of active timers */
static mame_timer timers[MAX_TIMERS];
static mame_timer *timer_head;
static mame_timer *timer_free_head;
static mame_timer *timer_free_tail;

/* other internal states */
static mame_time global_basetime;
static mame_timer *callback_timer;
static int callback_timer_modified;
static mame_time callback_timer_expire_time;

/* other constant times */
mame_time time_zero;
mame_time time_never;



/*-------------------------------------------------
	get_current_time - return the current time
-------------------------------------------------*/

INLINE mame_time get_current_time(void)
{
	int activecpu;

	/* if we're executing as a particular CPU, use its local time as a base */
	activecpu = cpu_getactivecpu();
	if (activecpu >= 0)
		return cpunum_get_localtime(activecpu);
	
	/* if we're currently in a callback, use the timer's expiration time as a base */
	if (callback_timer)
		return callback_timer_expire_time;
	
	/* otherwise, return the current global base time */
	return global_basetime;
}



/*-------------------------------------------------
	timer_new - allocate a new timer
-------------------------------------------------*/

INLINE mame_timer *timer_new(void)
{
	mame_timer *timer;

	/* remove an empty entry */
	if (!timer_free_head)
		return NULL;
	timer = timer_free_head;
	timer_free_head = timer->next;
	if (!timer_free_head)
		timer_free_tail = NULL;

	return timer;
}



/*-------------------------------------------------
	timer_list_insert - insert a new timer into
	the list at the appropriate location
-------------------------------------------------*/

INLINE void timer_list_insert(mame_timer *timer)
{
	mame_time expire = timer->enabled ? timer->expire : time_never;
	mame_timer *t, *lt = NULL;

	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		int tnum = 0;

		/* loop over the timer list */
		for (t = timer_head; t; t = t->next, tnum++)
		{
			if (t == timer)
				osd_die("This timer is already inserted in the list!\n");
			if (tnum == MAX_TIMERS-1)
				osd_die("Timer list is full!\n");
		}
	}
	#endif

	/* loop over the timer list */
	for (t = timer_head; t; lt = t, t = t->next)
	{
		/* if the current list entry expires after us, we should be inserted before it */
		if (compare_mame_times(t->expire, expire) > 0)
		{
			/* link the new guy in before the current list entry */
			timer->prev = t->prev;
			timer->next = t;

			if (t->prev)
				t->prev->next = timer;
			else
				timer_head = timer;
			t->prev = timer;
			return;
		}
	}

	/* need to insert after the last one */
	if (lt)
		lt->next = timer;
	else
		timer_head = timer;
	timer->prev = lt;
	timer->next = NULL;
}



/*-------------------------------------------------
	timer_list_remove - remove a timer from the
	linked list
-------------------------------------------------*/

INLINE void timer_list_remove(mame_timer *timer)
{
	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		mame_timer *t;
		int tnum = 0;

		/* loop over the timer list */
		for (t = timer_head; t && t != timer; t = t->next, tnum++) ;
		if (t == NULL)
			osd_die ("timer not found in list");
	}
	#endif

	/* remove it from the list */
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
}



/*-------------------------------------------------
	timer_init - initialize the timer system
-------------------------------------------------*/

void timer_init(void)
{
	int i;

	/* init the constant times */
	time_zero.seconds = time_zero.subseconds = 0;
	time_never.seconds = MAX_SECONDS;
	time_never.subseconds = MAX_SUBSECONDS - 1;

	/* we need to wait until the first call to timer_cyclestorun before using real CPU times */
	global_basetime = time_zero;
	callback_timer = NULL;
	callback_timer_modified = 0;

	/* reset the timers */
	memset(timers, 0, sizeof(timers));

	/* initialize the lists */
	timer_head = NULL;
	timer_free_head = &timers[0];
	for (i = 0; i < MAX_TIMERS-1; i++)
	{
		timers[i].tag = -1;
		timers[i].next = &timers[i+1];
	}
	timers[MAX_TIMERS-1].next = NULL;
	timer_free_tail = &timers[MAX_TIMERS-1];
}



/*-------------------------------------------------
	timer_free - remove all timers on the current
	resource tag
-------------------------------------------------*/

void timer_free(void)
{
	int tag = get_resource_tag();
	mame_timer *timer, *next;

	/* scan the list */
	for (timer = timer_head; timer != NULL; timer = next)
	{
		/* prefetch the next timer in case we remove this one */
		next = timer->next;

		/* if this tag matches, remove it */
		if (timer->tag == tag)
			timer_remove(timer);
	}
}



/*-------------------------------------------------
	mame_timer_next_fire_time - return the
	time when the next timer will fire
-------------------------------------------------*/

mame_time mame_timer_next_fire_time(void)
{
	return timer_head->expire;
}



/*-------------------------------------------------
	timer_adjust_global_time - adjust the global
	time; this is also where we fire the timers
-------------------------------------------------*/

void mame_timer_set_global_time(mame_time newbase)
{
	mame_timer *timer;

	/* set the new global offset */
	global_basetime = newbase;

	LOG(("mame_timer_set_global_time: new=%.9f head->expire=%.9f\n", mame_time_to_double(newbase), mame_time_to_double(timer_head->expire)));

	/* now process any timers that are overdue */
	while (compare_mame_times(timer_head->expire, global_basetime) <= 0)
	{
		int was_enabled = timer_head->enabled;

		/* if this is a one-shot timer, disable it now */
		timer = timer_head;
		if (compare_mame_times(timer->period, time_zero) == 0 || compare_mame_times(timer->period, time_never) == 0)
			timer->enabled = 0;

		/* set the global state of which callback we're in */
		callback_timer_modified = 0;
		callback_timer = timer;
		callback_timer_expire_time = timer->expire;

		/* call the callback */
		if (was_enabled && timer->callback)
		{
			LOG(("Timer %s:%d fired (expire=%.9f)\n", timer->file, timer->line, mame_time_to_double(timer->expire)));
			profiler_mark(PROFILER_TIMER_CALLBACK);
			(*timer->callback)(timer->callback_param);
			profiler_mark(PROFILER_END);
		}

		/* clear the callback timer global */
		callback_timer = NULL;

		/* reset or remove the timer, but only if it wasn't modified during the callback */
		if (!callback_timer_modified)
		{
			/* if the timer is temporary, remove it now */
			if (timer->temporary)
				timer_remove(timer);

			/* otherwise, reschedule it */
			else
			{
				timer->start = timer->expire;
				timer->expire = add_mame_times(timer->expire, timer->period);

				timer_list_remove(timer);
				timer_list_insert(timer);
			}
		}
	}
}



/*-------------------------------------------------
	timer_alloc - allocate a permament timer that
	isn't primed yet
-------------------------------------------------*/

mame_timer *_mame_timer_alloc(void (*callback)(int), const char *file, int line)
{
	mame_time time = get_current_time();
	mame_timer *timer = timer_new();

	/* fail if we can't allocate a new entry */
	if (!timer)
		return NULL;

	/* fill in the record */
	timer->callback = callback;
	timer->callback_param = 0;
	timer->enabled = 0;
	timer->temporary = 0;
	timer->tag = get_resource_tag();
	timer->period = time_zero;
	timer->file = file;
	timer->line = line;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = time_never;
	timer_list_insert(timer);

	/* return a handle */
	return timer;
}



/*-------------------------------------------------
	timer_adjust - adjust the time when this
	timer will fire, and whether or not it will
	fire periodically
-------------------------------------------------*/

void mame_timer_adjust(mame_timer *which, mame_time duration, int param, mame_time period)
{
	mame_time time = get_current_time();

	/* error if this is an inactive timer */
	if (which->tag == -1)
	{
		printf("mame_timer_adjust: adjusting an inactive timer!\n");
		logerror("mame_timer_adjust: adjusting an inactive timer!\n");
		return;
	}

	/* if this is the callback timer, mark it modified */
	if (which == callback_timer)
		callback_timer_modified = 1;

	/* compute the time of the next firing and insert into the list */
	which->callback_param = param;
	which->enabled = 1;

	/* set the start and expire times */
	which->start = time;
	which->expire = add_mame_times(time, duration);
	which->period = period;

	/* remove and re-insert the timer in its new order */
	timer_list_remove(which);
	timer_list_insert(which);

	/* if this was inserted as the head, abort the current timeslice and resync */
	LOG(("timer_adjust %s:%d to expire @ %.9f\n", which->file, which->line, mame_time_to_double(which->expire)));
	if (which == timer_head && cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*-------------------------------------------------
	timer_pulse - allocate a pulse timer, which
	repeatedly calls the callback using the given
	period
-------------------------------------------------*/

void _mame_timer_pulse(mame_time period, int param, void (*callback)(int), const char *file, int line)
{
	mame_timer *timer = _mame_timer_alloc(callback, file, line);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* adjust to our liking */
	mame_timer_adjust(timer, period, param, period);
}



/*-------------------------------------------------
	timer_set - allocate a one-shot timer, which
	calls the callback after the given duration
-------------------------------------------------*/

void _mame_timer_set(mame_time duration, int param, void (*callback)(int), const char *file, int line)
{
	mame_timer *timer = _mame_timer_alloc(callback, file, line);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* mark the timer temporary */
	timer->temporary = 1;

	/* adjust to our liking */
	mame_timer_adjust(timer, duration, param, time_zero);
}



/*-------------------------------------------------
	timer_reset - reset the timing on a timer
-------------------------------------------------*/

void mame_timer_reset(mame_timer *which, mame_time duration)
{
	/* error if this is an inactive timer */
	if (which->tag == -1)
	{
		printf("mame_timer_reset: resetting an inactive timer!\n");
		logerror("mame_timer_reset: resetting an inactive timer!\n");
		return;
	}

	/* adjust the timer */
	mame_timer_adjust(which, duration, which->callback_param, which->period);
}



/*-------------------------------------------------
	timer_remove - remove a timer from the system
-------------------------------------------------*/

void mame_timer_remove(mame_timer *which)
{
	/* error if this is an inactive timer */
	if (which->tag == -1)
	{
		printf("timer_remove: removing an inactive timer!\n");
		logerror("timer_remove: removed an inactive timer!\n");
		return;
	}

	/* remove it from the list */
	timer_list_remove(which);

	/* mark it as dead */
	which->tag = -1;

	/* free it up by adding it back to the free list */
	if (timer_free_tail)
		timer_free_tail->next = which;
	else
		timer_free_head = which;
	which->next = NULL;
	timer_free_tail = which;
}



/*-------------------------------------------------
	timer_enable - enable/disable a timer
-------------------------------------------------*/

int mame_timer_enable(mame_timer *which, int enable)
{
	int old;

	/* set the enable flag */
	old = which->enabled;
	which->enabled = enable;

	/* remove the timer and insert back into the list */
	timer_list_remove(which);
	timer_list_insert(which);

	return old;
}



/*-------------------------------------------------
	timer_timeelapsed - return the time since the
	last trigger
-------------------------------------------------*/

mame_time mame_timer_timeelapsed(mame_timer *which)
{
	return sub_mame_times(get_current_time(), which->start);
}



/*-------------------------------------------------
	timer_timeleft - return the time until the
	next trigger
-------------------------------------------------*/

mame_time mame_timer_timeleft(mame_timer *which)
{
	return sub_mame_times(which->expire, get_current_time());
}



/*-------------------------------------------------
	timer_get_time - return the current time
-------------------------------------------------*/

mame_time mame_timer_get_time(void)
{
	return get_current_time();
}



/*-------------------------------------------------
	timer_starttime - return the time when this
	timer started counting
-------------------------------------------------*/

mame_time mame_timer_starttime(mame_timer *which)
{
	return which->start;
}



/*-------------------------------------------------
	timer_firetime - return the time when this
	timer will fire next
-------------------------------------------------*/

mame_time mame_timer_firetime(mame_timer *which)
{
	return which->expire;
}
