/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

***************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------
    mame_time definitions
-------------------------------------------------*/

#define MAX_SUBSECONDS				((subseconds_t)1000000000 * (subseconds_t)1000000000)
#define MAX_SECONDS					((seconds_t)1000000000)

#define SUBSECONDS_TO_DOUBLE(x)		((double)(x) * (1.0 / (double)MAX_SUBSECONDS))
#define DOUBLE_TO_SUBSECONDS(x)		((subseconds_t)((x) * (double)MAX_SUBSECONDS))

typedef INT64 subseconds_t;
typedef INT32 seconds_t;

typedef struct
{
	seconds_t		seconds;
	subseconds_t	subseconds;
} mame_time;

extern mame_time time_zero;
extern mame_time time_never;

extern subseconds_t subseconds_per_cycle[];
extern UINT32 cycles_per_second[];

#define MAME_TIME_TO_CYCLES(cpu,t) ((t).seconds * cycles_per_second[cpu] + (t).subseconds / subseconds_per_cycle[cpu])
#define MAME_TIME_IN_CYCLES(c,cpu) (make_mame_time((c) / cycles_per_second[cpu], (c) * subseconds_per_cycle[cpu]))


/*-------------------------------------------------
    timing macros
-------------------------------------------------*/

extern double cycles_to_sec[];
extern double sec_to_cycles[];

#define TIME_IN_HZ(hz)        (1.0 / (double)(hz))
#define TIME_IN_CYCLES(c,cpu) ((double)(c) * cycles_to_sec[cpu])
#define TIME_IN_SEC(s)        ((double)(s))
#define TIME_IN_MSEC(ms)      ((double)(ms) * (1.0 / 1000.0))
#define TIME_IN_USEC(us)      ((double)(us) * (1.0 / 1000000.0))
#define TIME_IN_NSEC(us)      ((double)(us) * (1.0 / 1000000000.0))

// c must be greater then 1000pF to use this function
// r is in Ohms.  c is in Farads.
#define TIME_OF_74LS123(r,c)	(0.45 * (double)(r) * ((double)(c))

#define TIME_NOW              (0.0)
#define TIME_NEVER            (1.0e30)

#define TIME_TO_CYCLES(cpu,t) ((int)((t) * sec_to_cycles[cpu]))

typedef struct _mame_timer mame_timer;



/*-------------------------------------------------
    core functions
-------------------------------------------------*/

void timer_init(void);
void timer_free(void);

mame_time mame_timer_next_fire_time(void);
void mame_timer_set_global_time(mame_time newbase);
mame_timer *_mame_timer_alloc(void (*callback)(int), const char *file, int line);
mame_timer *_mame_timer_alloc_ptr(void (*callback)(void *), const char *file, int line);
void mame_timer_adjust(mame_timer *which, mame_time duration, int param, mame_time period);
void mame_timer_adjust_ptr(mame_timer *which, mame_time duration, void *param, mame_time period);
void _mame_timer_pulse(mame_time period, int param, void (*callback)(int), const char *file, int line);
void _mame_timer_pulse_ptr(mame_time period, void *param, void (*callback)(void *), const char *file, int line);
void _mame_timer_set(mame_time duration, int param, void (*callback)(int), const char *file, int line);
void _mame_timer_set_ptr(mame_time duration, void *param, void (*callback)(void *), const char *file, int line);
void mame_timer_reset(mame_timer *which, mame_time duration);
void mame_timer_remove(mame_timer *which);
int mame_timer_enable(mame_timer *which, int enable);
mame_time mame_timer_timeelapsed(mame_timer *which);
mame_time mame_timer_timeleft(mame_timer *which);
mame_time mame_timer_get_time(void);
mame_time mame_timer_starttime(mame_timer *which);
mame_time mame_timer_firetime(mame_timer *which);



/*-------------------------------------------------
    macros to pass debugging information
-------------------------------------------------*/

#define mame_timer_alloc(c)				_mame_timer_alloc(c, __FILE__, __LINE__)
#define mame_timer_alloc_ptr(c)			_mame_timer_alloc_ptr(c, __FILE__, __LINE__)
#define mame_timer_pulse(e,p,c)			_mame_timer_pulse(e, p, c, __FILE__, __LINE__)
#define mame_timer_pulse_ptr(e,p,c)		_mame_timer_pulse_ptr(e, p, c, __FILE__, __LINE__)
#define mame_timer_set(d,p,c)			_mame_timer_set(d, p, c, __FILE__, __LINE__)
#define mame_timer_set_ptr(d,p,c)		_mame_timer_set_ptr(d, p, c, __FILE__, __LINE__)



/*-------------------------------------------------
    old functions mapped to new ones
-------------------------------------------------*/

#define timer_alloc(c)					mame_timer_alloc(c)
#define timer_alloc_ptr(c)				mame_timer_alloc_ptr(c)
#define timer_adjust(w,d,p,e)			mame_timer_adjust(w, double_to_mame_time(d), p, double_to_mame_time(e))
#define timer_adjust_ptr(w,d,p,e)		mame_timer_adjust_ptr(w, double_to_mame_time(d), p, double_to_mame_time(e))
#define timer_pulse(e,p,c)				mame_timer_pulse(double_to_mame_time(e), p, c)
#define timer_pulse_ptr(e,p,c)			mame_timer_pulse_ptr(double_to_mame_time(e), p, c)
#define timer_set(d,p,c)				mame_timer_set(double_to_mame_time(d), p, c)
#define timer_set_ptr(d,p,c)			mame_timer_set_ptr(double_to_mame_time(d), p, c)
#define timer_reset(w,d)				mame_timer_reset(w, double_to_mame_time(d))
#define timer_remove(w)					mame_timer_remove(w)
#define timer_enable(w,e)				mame_timer_enable(w,e)
#define timer_timeelapsed(w)			mame_time_to_double(mame_timer_timeelapsed(w))
#define timer_timeleft(w)				mame_time_to_double(mame_timer_timeleft(w))
#define timer_get_time()				mame_time_to_double(mame_timer_get_time())
#define timer_starttime(w)				mame_time_to_double(mame_timer_starttime(w))
#define timer_firetime(w)				mame_time_to_double(mame_timer_firetime(w))



/*-------------------------------------------------
    make a MAME time
-------------------------------------------------*/

INLINE mame_time make_mame_time(seconds_t _secs, subseconds_t _subsecs)
{
	mame_time result;
	result.seconds = _secs;
	result.subseconds = _subsecs;
	return result;
}


/*-------------------------------------------------
    convert from mame_time to double
-------------------------------------------------*/

INLINE double mame_time_to_double(mame_time _time)
{
	return (double)_time.seconds + SUBSECONDS_TO_DOUBLE(_time.subseconds);
}


/*-------------------------------------------------
    convert from double to mame_time
-------------------------------------------------*/

INLINE mame_time double_to_mame_time(double _time)
{
	mame_time abstime;

	/* special case for TIME_NEVER */
	if (_time >= TIME_NEVER)
		return time_never;

	/* set seconds to the integral part */
	abstime.seconds = (INT64)_time;

	/* set subseconds to the fractional part */
	_time -= (double)abstime.seconds;
	abstime.subseconds = DOUBLE_TO_SUBSECONDS(_time);
	return abstime;
}


/*-------------------------------------------------
    add two mame_times
-------------------------------------------------*/

INLINE mame_time add_mame_times(mame_time _time1, mame_time _time2)
{
	mame_time result;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS || _time2.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds + _time2.subseconds;
	result.seconds = _time1.seconds + _time2.seconds;

	/* normalize and return */
	if (result.subseconds >= MAX_SUBSECONDS)
	{
		result.subseconds -= MAX_SUBSECONDS;
		result.seconds++;
	}
	return result;
}


/*-------------------------------------------------
    add subseconds to a mame_time
-------------------------------------------------*/

INLINE mame_time add_subseconds_to_mame_time(mame_time _time1, subseconds_t _subseconds)
{
	mame_time result;

	/* if one of the items is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds + _subseconds;
	result.seconds = _time1.seconds;

	/* normalize and return */
	if (result.subseconds >= MAX_SUBSECONDS)
	{
		result.subseconds -= MAX_SUBSECONDS;
		result.seconds++;
	}
	return result;
}


/*-------------------------------------------------
    subtract two mame_times
-------------------------------------------------*/

INLINE mame_time sub_mame_times(mame_time _time1, mame_time _time2)
{
	mame_time result;

	/* if time1 is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds - _time2.subseconds;
	result.seconds = _time1.seconds - _time2.seconds;

	/* normalize and return */
	if (result.subseconds < 0)
	{
		result.subseconds += MAX_SUBSECONDS;
		result.seconds--;
	}
	return result;
}


/*-------------------------------------------------
    subtract subseconds from a mame_time
-------------------------------------------------*/

INLINE mame_time sub_subseconds_from_mame_time(mame_time _time1, subseconds_t _subseconds)
{
	mame_time result;

	/* if time1 is time_never, return time_never */
	if (_time1.seconds >= MAX_SECONDS)
		return time_never;

	/* add the seconds and subseconds */
	result.subseconds = _time1.subseconds - _subseconds;
	result.seconds = _time1.seconds;

	/* normalize and return */
	if (result.subseconds < 0)
	{
		result.subseconds += MAX_SUBSECONDS;
		result.seconds--;
	}
	return result;
}


/*-------------------------------------------------
    compare two mame_times
-------------------------------------------------*/

INLINE int compare_mame_times(mame_time _time1, mame_time _time2)
{
	if (_time1.seconds > _time2.seconds)
		return 1;
	if (_time1.seconds < _time2.seconds)
		return -1;
	if (_time1.subseconds > _time2.subseconds)
		return 1;
	if (_time1.subseconds < _time2.subseconds)
		return -1;
	return 0;
}


#ifdef __cplusplus
}
#endif

#endif
