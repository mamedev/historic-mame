#include "driver.h"

int use_profiler;
unsigned int curr_cycles;

void my_textout(char *buf,int x,int y);

#define MEMORY 6

struct profile_data
{
	unsigned int count[MEMORY][OSD_TOTAL_PROFILES];
	unsigned int cpu_context_switches[MEMORY];
};

static struct profile_data profile;
static int memory;


void osd_profiler(int type)
{
	static int FILO_type[10];
	static int FILO_start[10];
	static int FILO_length;


	if (!use_profiler) return;

	if (type >= OSD_PROFILE_CPU1 && type <= OSD_PROFILE_CPU4)
		profile.cpu_context_switches[memory]++;

	__asm__ (
		"pushl %eax            \n"
		"pushl %edx            \n"
		"rdtsc                 \n"
		"movl %eax, _curr_cycles \n"
//		"movl %edx, _curr_cycles_high \n"
		"popl %edx             \n"
		"popl %eax             \n");

	if (type != OSD_PROFILE_END)
	{
		if (FILO_length >= 10)
		{
if (errorlog) fprintf(errorlog,"Profiler error: FILO buffer overflow\n");
			return;
		}

		if (FILO_length > 0)
		{
			/* handle nested calls */
			profile.count[memory][FILO_type[FILO_length-1]] += curr_cycles - FILO_start[FILO_length-1];
		}
		FILO_type[FILO_length] = type;
		FILO_start[FILO_length] = curr_cycles;
		FILO_length++;
	}
	else
	{
		if (FILO_length <= 0)
		{
if (errorlog) fprintf(errorlog,"Profiler error: FILO buffer underflow\n");
			return;
		}

		profile.count[memory][FILO_type[FILO_length-1]] += curr_cycles - FILO_start[FILO_length-1];
		FILO_length--;
		if (FILO_length > 0)
		{
			/* handle nested calls */
			FILO_start[FILO_length-1] = curr_cycles;
		}
	}
}

void osd_profiler_display(void)
{
	int i,j;
	unsigned int total,normalize;
	unsigned int computed;
	int line;
	char buf[30];
	static char *names[OSD_TOTAL_PROFILES] =
	{
		"CPU 1",
		"CPU 2",
		"CPU 3",
		"CPU 4",
		"Video",
		"Blit ",
		"Sound",
		"Cllbk",
		"Extra",
		"User1",
		"User2",
		"User3",
		"User4",
		"Prflr",
		"Idle ",
	};


	if (!use_profiler) return;

	osd_profiler(OSD_PROFILE_PROFILER);

	computed = 0;
	i = 0;
	while (i < OSD_PROFILE_PROFILER)
	{
		for (j = 0;j < MEMORY;j++)
			computed += profile.count[j][i];
		i++;
	}
	normalize = computed;
	while (i < OSD_TOTAL_PROFILES)
	{
		for (j = 0;j < MEMORY;j++)
			computed += profile.count[j][i];
		i++;
	}
	total = computed;

	if (total == 0 || normalize == 0) return;	/* we have been just reset */

	line = 0;
	for (i = 0;i < OSD_TOTAL_PROFILES;i++)
	{
		computed = 0;
		{
			for (j = 0;j < MEMORY;j++)
				computed += profile.count[j][i];
		}
		if (computed)
		{
			if (i < OSD_PROFILE_PROFILER)
				sprintf(buf,"%s%3d%%%3d%%",names[i],(computed + total/200) / (total/100),(computed + normalize/200) / (normalize/100));
			else
				sprintf(buf,"%s%3d%%",names[i],(computed + total/200) / (total/100));
			my_textout(buf,0,(line++)*Machine->uifont->height);
		}
	}

	computed = 0;
	{
		for (j = 0;j < MEMORY;j++)
			computed += profile.cpu_context_switches[j];
	}
	sprintf(buf,"CPU switches%4d",computed / MEMORY);
	my_textout(buf,0,(line++)*Machine->uifont->height);

	/* reset the counters */
	memory = (memory + 1) % MEMORY;
	profile.cpu_context_switches[memory] = 0;
	for (i = 0;i < OSD_TOTAL_PROFILES;i++)
		profile.count[memory][i] = 0;

	osd_profiler(OSD_PROFILE_END);
}
