//============================================================
//
//	ticker.c - Win32 timing code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// MAME headers
#include "driver.h"



//============================================================
//	PROTOTYPES
//============================================================

static cycles_t init_cycle_counter(void);
static cycles_t rdtsc_cycle_counter(void);
static cycles_t time_cycle_counter(void);



//============================================================
//	GLOBAL VARIABLES
//============================================================

// global cycle_counter function and divider
cycles_t		(*cycle_counter)(void) = init_cycle_counter;
cycles_t		cycles_per_sec;



//============================================================
//	init_cycle_counter
//============================================================

#ifdef _MSC_VER

static int has_rdtsc(void)
{
	int nFeatures;

	__asm {

		mov eax, 1
		cpuid
		mov nFeatures, edx
	}

	return ((nFeatures & 0x10) == 0x10) ? TRUE : FALSE;
}

#else

static int has_rdtsc(void)
{
	int result;

	__asm__ (
		"movl $1,%%eax     ; "
		"xorl %%ebx,%%ebx  ; "
		"xorl %%ecx,%%ecx  ; "
		"xorl %%edx,%%edx  ; "
		"cpuid             ; "
		"testl $0x10,%%edx ; "
		"setne %%al        ; "
		"andl $1,%%eax     ; "
	:  "=&a" (result)   /* the result has to go in eax */
	:       /* no inputs */
	:  "%ebx", "%ecx", "%edx" /* clobbers ebx ecx edx */
	);
	return result;
}

#endif



//============================================================
//	init_cycle_counter
//============================================================

static cycles_t init_cycle_counter(void)
{
	cycles_t start, end;
	DWORD a, b;
	int priority;

	// if the RDTSC instruction is available use it because
	// it is more precise and has less overhead than timeGetTime()
	if (has_rdtsc())
	{
		cycle_counter = rdtsc_cycle_counter;
		logerror("using RDTSC for timing ... ");
	}
	else
	{
		cycle_counter = time_cycle_counter;
		logerror("using timeGetTime for timing ... ");
	}

	// temporarily set our priority higher
	priority = GetThreadPriority(GetCurrentThread());
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

	// reduce our priority
	SetThreadPriority(GetCurrentThread(), priority);

	// log the results
	logerror("cycles/second = %d\n", (int)cycles_per_sec);

	// return the current cycle count
	return (*cycle_counter)();
}



//============================================================
//	rdtsc_cycle_counter
//============================================================

#ifdef _MSC_VER

static cycles_t rdtsc_cycle_counter(void)
{
	INT64 result;
	INT64 *presult = &result;

	__asm {

		rdtsc
		mov ebx, presult
		mov [ebx],eax
		mov [ebx+4],edx
	}

	return result;
}

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
//	time_cycle_counter
//============================================================

static cycles_t time_cycle_counter(void)
{
	// use timeGetTime
	return (cycles_t)timeGetTime();
}



//============================================================
//	osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return (*cycle_counter)();
}



//============================================================
//	osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return cycles_per_sec;
}
