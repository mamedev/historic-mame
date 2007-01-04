//============================================================
//
//  ticker.c - Win32 timing code
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "x86drc.h"
#include "options.h"
#include "video.h"



//============================================================
//  PROTOTYPES
//============================================================

static cycles_t init_cycle_counter(void);
static cycles_t performance_cycle_counter(void);
static cycles_t rdtsc_cycle_counter(void);
static cycles_t time_cycle_counter(void);
static cycles_t nop_cycle_counter(void);



//============================================================
//  GLOBAL VARIABLES
//============================================================

// global cycle_counter function and divider
cycles_t		(*cycle_counter)(void) = init_cycle_counter;
cycles_t		(*ticks_counter)(void) = init_cycle_counter;
cycles_t		cycles_per_sec;



//============================================================
//  STATIC VARIABLES
//============================================================

static cycles_t suspend_adjustment;
static cycles_t suspend_time;


//============================================================
//  init_cycle_counter
//============================================================

static cycles_t init_cycle_counter(void)
{
	cycles_t start, end;
	DWORD a, b;
	int priority = GetThreadPriority(GetCurrentThread());
	LARGE_INTEGER frequency;

	suspend_adjustment = 0;
	suspend_time = 0;

	if (!options_get_bool("rdtsc") && QueryPerformanceFrequency( &frequency ))
	{
		// use performance counter if available as it is constant
		cycle_counter = performance_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
		logerror("using performance counter for timing ... ");

		cycles_per_sec = frequency.QuadPart;
	}
	else
	{
		// if the RDTSC instruction is available use it because
		// it is more precise and has less overhead than timeGetTime()
		cycle_counter = rdtsc_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
		logerror("using RDTSC for timing ... ");

		// temporarily set our priority higher
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		// wait for an edge on the timeGetTime call
		a = timeGetTime();
		do
		{
			b = timeGetTime();
		} while (a == b);

		// get the starting cycle count
		start = (*cycle_counter)();

		// now wait for 1/4 second total
		do
		{
			a = timeGetTime();
		} while (a - b < 250);

		// get the ending cycle count
		end = (*cycle_counter)();

		// compute ticks_per_sec
		cycles_per_sec = (end - start) * 4;
	}

	// restore our priority
	SetThreadPriority(GetCurrentThread(), priority);

	// log the results
	logerror("cycles/second = %u\n", (int)cycles_per_sec);
	logerror("thread priority = %d\n", priority );

	// return the current cycle count
	return (*cycle_counter)();
}



//============================================================
//  performance_cycle_counter
//============================================================

static cycles_t performance_cycle_counter(void)
{
	LARGE_INTEGER performance_count;
	QueryPerformanceCounter(&performance_count);
	return (cycles_t)performance_count.QuadPart;
}



//============================================================
//  rdtsc_cycle_counter
//============================================================

#ifdef _MSC_VER

#ifdef PTR64

static cycles_t rdtsc_cycle_counter(void)
{
	return __rdtsc();
}

#else

static cycles_t rdtsc_cycle_counter(void)
{
	INT64 result;
	INT64 *presult = &result;

	__asm {
		__asm _emit 0Fh __asm _emit 031h	// rdtsc
		mov ebx, presult
		mov [ebx],eax
		mov [ebx+4],edx
	}

	return result;
}

#endif

#else

static cycles_t rdtsc_cycle_counter(void)
{
	INT64 result;

	// use RDTSC
	__asm__ __volatile__ (
		"rdtsc"
		: "=A" (result)
	);

	return result;
}

#endif



//============================================================
//  time_cycle_counter
//============================================================

static cycles_t time_cycle_counter(void)
{
	// use timeGetTime
	return (cycles_t)timeGetTime();
}



//============================================================
//  nop_cycle_counter
//============================================================

static cycles_t nop_cycle_counter(void)
{
	return 0;
}



//============================================================
//  osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return suspend_time ? suspend_time : (*cycle_counter)() - suspend_adjustment;
}



//============================================================
//  osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return cycles_per_sec;
}



//============================================================
//  osd_profiling_ticks
//============================================================

cycles_t osd_profiling_ticks(void)
{
	return (*ticks_counter)();
}



//============================================================
//  win_timer_enable
//============================================================

void win_timer_enable(int enabled)
{
	cycles_t actual_cycles;

	actual_cycles = (*cycle_counter)();
	if (!enabled)
	{
		suspend_time = actual_cycles;
	}
	else if (suspend_time > 0)
	{
		suspend_adjustment += actual_cycles - suspend_time;
		suspend_time = 0;
	}
}

